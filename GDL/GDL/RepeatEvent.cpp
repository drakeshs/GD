/** \file
 *  Game Develop
 *  2008-2011 Florian Rival (Florian.Rival@gmail.com)
 */
#include "RepeatEvent.h"
#include "RuntimeScene.h"
#include "ObjectsConcerned.h"
#include "GDL/OpenSaveGame.h"

#if defined(GD_IDE_ONLY)
#include "GDL/EventsRenderingHelper.h"
#include "GDL/EditRepeatEvent.h"
#endif

RepeatEvent::RepeatEvent() :
BaseEvent(),
repeatNumberExpression("")
#if defined(GD_IDE_ONLY)
,repeatNumberExpressionSelected(false)
#endif
{
}

/**
 * Check the conditions, and launch actions and subevents if necessary
 */
void RepeatEvent::Execute( RuntimeScene & scene, ObjectsConcerned & objectsConcerned )
{
    int nbRepeat = repeatNumberExpression.GetAsMathExpressionResult(scene, objectsConcerned);

    for (unsigned int r = 0;r<static_cast<unsigned>(nbRepeat);++r)
    {
        ObjectsConcerned objectsConcernedForEvent;
        objectsConcernedForEvent.InheritsFrom(&objectsConcerned);

        if ( ExecuteConditions( scene, objectsConcernedForEvent) == true )
        {
            ExecuteActions( scene, objectsConcernedForEvent);

            for (unsigned int i = 0;i<events.size();++i)
            {
                ObjectsConcerned objectsConcernedForSubEvent;
                objectsConcernedForSubEvent.InheritsFrom(&objectsConcernedForEvent);

                events[i]->Execute(scene, objectsConcernedForSubEvent);
            }
        }
    }
}

/**
 * Check if all conditions are true
 */
bool RepeatEvent::ExecuteConditions( RuntimeScene & scene, ObjectsConcerned & objectsConcerned )
{
    for ( unsigned int k = 0; k < conditions.size(); ++k )
    {
        if ( conditions[k].function != NULL &&
             !conditions[k].function( scene, objectsConcerned, conditions[k]) )
            return false; //Return false as soon as a condition is false
    }

    return true;
}

/**
 * Run actions of the event
 */
void RepeatEvent::ExecuteActions( RuntimeScene & scene, ObjectsConcerned & objectsConcerned )
{
    for ( unsigned int k = 0; k < actions.size();k++ )
    {
        if ( actions[k].function != NULL )
            actions[k].function( scene, objectsConcerned, actions[k]);
    }

    return;
}

vector < vector<Instruction>* > RepeatEvent::GetAllConditionsVectors()
{
    vector < vector<Instruction>* > allConditions;
    allConditions.push_back(&conditions);

    return allConditions;
}

vector < vector<Instruction>* > RepeatEvent::GetAllActionsVectors()
{
    vector < vector<Instruction>* > allActions;
    allActions.push_back(&actions);

    return allActions;
}

vector < GDExpression* > RepeatEvent::GetAllExpressions()
{
    vector < GDExpression* > allExpressions;
    allExpressions.push_back(&repeatNumberExpression);

    return allExpressions;
}

#if defined(GD_IDE_ONLY)
void RepeatEvent::SaveToXml(TiXmlElement * eventElem) const
{
    TiXmlElement * repeatElem = new TiXmlElement( "RepeatExpression" );
    eventElem->LinkEndChild( repeatElem );
    repeatElem->SetAttribute("value", repeatNumberExpression.GetPlainString().c_str());

    //Les conditions
    TiXmlElement * conditionsElem = new TiXmlElement( "Conditions" );
    eventElem->LinkEndChild( conditionsElem );
    OpenSaveGame::SaveConditions(conditions, conditionsElem);

    //Les actions
    TiXmlElement * actionsElem = new TiXmlElement( "Actions" );
    eventElem->LinkEndChild( actionsElem );
    OpenSaveGame::SaveActions(actions, actionsElem);

    //Sous �v�nements
    if ( !GetSubEvents().empty() )
    {
        TiXmlElement * subeventsElem;
        subeventsElem = new TiXmlElement( "Events" );
        eventElem->LinkEndChild( subeventsElem );

        OpenSaveGame::SaveEvents(events, subeventsElem);
    }
}
#endif

void RepeatEvent::LoadFromXml(const TiXmlElement * eventElem)
{
    if ( eventElem->FirstChildElement( "RepeatExpression" ) != NULL )
        repeatNumberExpression = GDExpression(eventElem->FirstChildElement("RepeatExpression")->Attribute("value"));

    //Conditions
    if ( eventElem->FirstChildElement( "Conditions" ) != NULL )
        OpenSaveGame::OpenConditions(conditions, eventElem->FirstChildElement( "Conditions" ));
    else
        cout << "Aucune informations sur les conditions d'un �v�nement";

    //Actions
    if ( eventElem->FirstChildElement( "Actions" ) != NULL )
        OpenSaveGame::OpenActions(actions, eventElem->FirstChildElement( "Actions" ));
    else
        cout << "Aucune informations sur les actions d'un �v�nement";

    //Subevents
    if ( eventElem->FirstChildElement( "Events" ) != NULL )
        OpenSaveGame::OpenEvents(events, eventElem->FirstChildElement( "Events" ));
}

#if defined(GD_IDE_ONLY)
void RepeatEvent::OnSingleClick(int x, int y, vector < boost::tuple< vector < BaseEventSPtr > *, unsigned int, vector < Instruction > *, unsigned int > > & eventsSelected,
                         bool & conditionsSelected, bool & instructionsSelected)
{
    const int repeatTextHeight = 20;
    EventsRenderingHelper * renderingHelper = EventsRenderingHelper::GetInstance();

    //Test selection for the "Repeat x times"
    if ( y >= 0 && y <= repeatTextHeight )
    {
        repeatNumberExpressionSelected = true;
        return;
    }

    //Test selection of actions/conditions
    repeatNumberExpressionSelected = false;
    y -= repeatTextHeight; //Substract the height of the "Repeat x times" text so as to simplify the tests
    if ( x <= renderingHelper->GetConditionsColumnWidth())
    {
        conditionsSelected = true;

        vector < Instruction > * conditionsListSelected = NULL;
        unsigned int conditionIdInList = 0;

        bool found = renderingHelper->GetConditionAt(conditions, x-0, y-0, conditionsListSelected, conditionIdInList);

        if ( found )
        {
            //Update event and conditions selection information
            if ( conditionIdInList < conditionsListSelected->size() ) (*conditionsListSelected)[conditionIdInList].selected = true;

            //Update editor selection information
            instructionsSelected = true;
            boost::tuples::get<2>(eventsSelected.back()) = conditionsListSelected;
            boost::tuples::get<3>(eventsSelected.back()) = conditionIdInList;

            return;
        }
        else if ( y <= 18 )
        {

            //Update selection information
            instructionsSelected = true;
            boost::tuples::get<2>(eventsSelected.back()) = &conditions;
            boost::tuples::get<3>(eventsSelected.back()) = 0;

            return;
        }
    }
    else
    {
        conditionsSelected = false;

        vector < Instruction > * actionsListSelected = NULL;
        unsigned int actionIdInList = 0;

        bool found = renderingHelper->GetActionAt(actions, x-0, y-0, actionsListSelected, actionIdInList);

        if ( found )
        {
            //Update event and action selection information
            if ( actionIdInList < actionsListSelected->size() ) (*actionsListSelected)[actionIdInList].selected = true;

            //Update selection information
            instructionsSelected = true;
            boost::tuples::get<2>(eventsSelected.back()) = actionsListSelected;
            boost::tuples::get<3>(eventsSelected.back()) = actionIdInList;
        }
        else
        {
            //Update selection information
            instructionsSelected = true;
            boost::tuples::get<2>(eventsSelected.back()) = &actions;
            boost::tuples::get<3>(eventsSelected.back()) = 0;
        }
    }
}

/**
 * Render the event in the bitmap
 */
void RepeatEvent::Render(wxBufferedPaintDC & dc, int x, int y, unsigned int width) const
{
    EventsRenderingHelper * renderingHelper = EventsRenderingHelper::GetInstance();
    const int repeatTextHeight = 20;

    //Draw event rectangle
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(wxColour(255, 255, 255), wxBRUSHSTYLE_SOLID));
    {
        wxRect rect(x, y, width, GetRenderedHeight(width));
        wxColor color1 = selected ? renderingHelper->selectionColor : (IsDisabled() ? renderingHelper->disabledColor2 :renderingHelper->eventGradient1);
        wxColor color2 = IsDisabled() ? renderingHelper->disabledColor : renderingHelper->eventGradient2;
        wxColor color3 = IsDisabled() ? renderingHelper->disabledColor : renderingHelper->eventGradient3;
        wxColor color4 = selected ? renderingHelper->selectionColor : (IsDisabled() ? renderingHelper->disabledColor2 :renderingHelper->eventGradient4);

        renderingHelper->DrawNiceRectangle(dc, rect, color1, color2, color3, color4, renderingHelper->eventBorderColor);
    }

    //"Repeat" text selection
    if ( selected && repeatNumberExpressionSelected )
    {
        dc.SetBrush(renderingHelper->GetSelectedRectangleFillBrush());
        dc.SetPen(renderingHelper->GetSelectedRectangleOutlinePen());
        dc.DrawRectangle(x+1, y+1, width-2, repeatTextHeight-2);
    }

    //Repeat text
    dc.SetFont( renderingHelper->GetBoldFont() );
    dc.DrawText( _T("R�p�ter") + " " + repeatNumberExpression.GetPlainString() + " " + _T("fois :"), x + 2, y + 1 );

    //Draw actions and conditions
    renderingHelper->DrawConditionsList(conditions, dc, x, y+repeatTextHeight, renderingHelper->GetConditionsColumnWidth(), IsDisabled());
    renderingHelper->DrawActionsList(actions, dc, x+renderingHelper->GetConditionsColumnWidth(), y+repeatTextHeight, width-renderingHelper->GetConditionsColumnWidth(), IsDisabled());
}

unsigned int RepeatEvent::GetRenderedHeight(unsigned int width) const
{
    if ( eventHeightNeedUpdate )
    {
        EventsRenderingHelper * renderingHelper = EventsRenderingHelper::GetInstance();
        const int repeatTextHeight = 20;

        //Get maximum height needed
        int conditionsHeight = renderingHelper->GetRenderedConditionsListHeight(conditions, renderingHelper->GetConditionsColumnWidth());
        int actionsHeight = renderingHelper->GetRenderedActionsListHeight(actions, width-renderingHelper->GetConditionsColumnWidth());

        renderedHeight = (( conditionsHeight > actionsHeight ? conditionsHeight : actionsHeight ) + repeatTextHeight);
        eventHeightNeedUpdate = false;
    }

    return renderedHeight;
}

void RepeatEvent::EditEvent(wxWindow* parent_, Game & game_, Scene & scene_, MainEditorCommand & mainEditorCommand_)
{
    EditRepeatEvent dialog(parent_, *this, game_, scene_);
    dialog.ShowModal();
}
#endif

/**
 * Initialize from another RepeatEvent.
 * Used by copy ctor and assignement operator
 */
void RepeatEvent::Init(const RepeatEvent & event)
{
    events = CloneVectorOfEvents(event.events);

    conditions = event.conditions;
    actions = event.actions;
    repeatNumberExpression = event.repeatNumberExpression;
}

/**
 * Custom copy operator
 */
RepeatEvent::RepeatEvent(const RepeatEvent & event) :
BaseEvent(event),
repeatNumberExpression("")
{
    Init(event);
}

/**
 * Custom assignement operator
 */
RepeatEvent& RepeatEvent::operator=(const RepeatEvent & event)
{
    if ( this != &event )
    {
        BaseEvent::operator=(event);
        Init(event);
    }

    return *this;
}
